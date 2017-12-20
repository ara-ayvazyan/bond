
#pragma once

#include "alias_with_allocator_types.h"
#include <bond/core/reflection.h>

namespace test
{
    //
    // foo
    //
    struct foo::Schema
    {
        typedef ::bond::no_base base;

        static const ::bond::Metadata metadata;
        
        private: static const ::bond::Metadata s_l_metadata;
        private: static const ::bond::Metadata s_v_metadata;
        private: static const ::bond::Metadata s_s_metadata;
        private: static const ::bond::Metadata s_m_metadata;
        private: static const ::bond::Metadata s_st_metadata;
        private: static const ::bond::Metadata s_d_metadata;
        private: static const ::bond::Metadata s_l1_metadata;
        private: static const ::bond::Metadata s_v1_metadata;
        private: static const ::bond::Metadata s_s1_metadata;
        private: static const ::bond::Metadata s_m1_metadata;
        private: static const ::bond::Metadata s_st1_metadata;
        private: static const ::bond::Metadata s_na_metadata;

        public: struct var
        {
            // l
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                foo,
                std::list<bool, typename std::allocator_traits<arena>::template rebind_alloc<bool> >,
                &foo::l,
                &s_l_metadata
            > {} l;
        
            // v
            typedef struct : ::bond::reflection::FieldTemplate<
                1,
                1,
                ::bond::reflection::optional_field_modifier,
                foo,
                std::vector<bool, typename std::allocator_traits<arena>::template rebind_alloc<bool> >,
                &foo::v,
                &s_v_metadata
            > {} v;
        
            // s
            typedef struct : ::bond::reflection::FieldTemplate<
                2,
                2,
                ::bond::reflection::optional_field_modifier,
                foo,
                std::set<bool, std::less<bool>, typename std::allocator_traits<arena>::template rebind_alloc<bool> >,
                &foo::s,
                &s_s_metadata
            > {} s;
        
            // m
            typedef struct : ::bond::reflection::FieldTemplate<
                3,
                3,
                ::bond::reflection::optional_field_modifier,
                foo,
                std::map<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >, bool, std::less<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> > >, typename std::allocator_traits<arena>::template rebind_alloc<std::pair<const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >, bool> > >,
                &foo::m,
                &s_m_metadata
            > {} m;
        
            // st
            typedef struct : ::bond::reflection::FieldTemplate<
                4,
                4,
                ::bond::reflection::optional_field_modifier,
                foo,
                std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >,
                &foo::st,
                &s_st_metadata
            > {} st;
        
            // d
            typedef struct : ::bond::reflection::FieldTemplate<
                5,
                5,
                ::bond::reflection::optional_field_modifier,
                foo,
                std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >,
                &foo::d,
                &s_d_metadata
            > {} d;
        
            // l1
            typedef struct : ::bond::reflection::FieldTemplate<
                10,
                6,
                ::bond::reflection::optional_field_modifier,
                foo,
                ::bond::maybe<std::list<bool, typename std::allocator_traits<arena>::template rebind_alloc<bool> > >,
                &foo::l1,
                &s_l1_metadata
            > {} l1;
        
            // v1
            typedef struct : ::bond::reflection::FieldTemplate<
                11,
                7,
                ::bond::reflection::optional_field_modifier,
                foo,
                ::bond::maybe<std::vector<bool, typename std::allocator_traits<arena>::template rebind_alloc<bool> > >,
                &foo::v1,
                &s_v1_metadata
            > {} v1;
        
            // s1
            typedef struct : ::bond::reflection::FieldTemplate<
                12,
                8,
                ::bond::reflection::optional_field_modifier,
                foo,
                ::bond::maybe<std::set<bool, std::less<bool>, typename std::allocator_traits<arena>::template rebind_alloc<bool> > >,
                &foo::s1,
                &s_s1_metadata
            > {} s1;
        
            // m1
            typedef struct : ::bond::reflection::FieldTemplate<
                13,
                9,
                ::bond::reflection::optional_field_modifier,
                foo,
                ::bond::maybe<std::map<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >, bool, std::less<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> > >, typename std::allocator_traits<arena>::template rebind_alloc<std::pair<const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >, bool> > > >,
                &foo::m1,
                &s_m1_metadata
            > {} m1;
        
            // st1
            typedef struct : ::bond::reflection::FieldTemplate<
                14,
                10,
                ::bond::reflection::optional_field_modifier,
                foo,
                ::bond::maybe<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> > >,
                &foo::st1,
                &s_st1_metadata
            > {} st1;
        
            // na
            typedef struct : ::bond::reflection::FieldTemplate<
                15,
                11,
                ::bond::reflection::optional_field_modifier,
                foo,
                std::set<std::list<std::map<int32_t, std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >, std::less<int32_t>, typename std::allocator_traits<arena>::template rebind_alloc<std::pair<const int32_t, std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> > > > >, typename std::allocator_traits<arena>::template rebind_alloc<std::map<int32_t, std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >, std::less<int32_t>, typename std::allocator_traits<arena>::template rebind_alloc<std::pair<const int32_t, std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> > > > > > >, std::less<std::list<std::map<int32_t, std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >, std::less<int32_t>, typename std::allocator_traits<arena>::template rebind_alloc<std::pair<const int32_t, std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> > > > >, typename std::allocator_traits<arena>::template rebind_alloc<std::map<int32_t, std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >, std::less<int32_t>, typename std::allocator_traits<arena>::template rebind_alloc<std::pair<const int32_t, std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> > > > > > > >, typename std::allocator_traits<arena>::template rebind_alloc<std::list<std::map<int32_t, std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >, std::less<int32_t>, typename std::allocator_traits<arena>::template rebind_alloc<std::pair<const int32_t, std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> > > > >, typename std::allocator_traits<arena>::template rebind_alloc<std::map<int32_t, std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >, std::less<int32_t>, typename std::allocator_traits<arena>::template rebind_alloc<std::pair<const int32_t, std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> > > > > > > > >,
                &foo::na,
                &s_na_metadata
            > {} na;
        };

        using field_count = std::integral_constant<uint16_t, 12>;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4348) // VC bug: redefinition of default parameter
#endif
        template <uint16_t I, int = 0> struct field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<var::l> {};
        template <int __bond_dummy> struct field<1, __bond_dummy> : ::bond::detail::mpl::identity<var::v> {};
        template <int __bond_dummy> struct field<2, __bond_dummy> : ::bond::detail::mpl::identity<var::s> {};
        template <int __bond_dummy> struct field<3, __bond_dummy> : ::bond::detail::mpl::identity<var::m> {};
        template <int __bond_dummy> struct field<4, __bond_dummy> : ::bond::detail::mpl::identity<var::st> {};
        template <int __bond_dummy> struct field<5, __bond_dummy> : ::bond::detail::mpl::identity<var::d> {};
        template <int __bond_dummy> struct field<6, __bond_dummy> : ::bond::detail::mpl::identity<var::l1> {};
        template <int __bond_dummy> struct field<7, __bond_dummy> : ::bond::detail::mpl::identity<var::v1> {};
        template <int __bond_dummy> struct field<8, __bond_dummy> : ::bond::detail::mpl::identity<var::s1> {};
        template <int __bond_dummy> struct field<9, __bond_dummy> : ::bond::detail::mpl::identity<var::m1> {};
        template <int __bond_dummy> struct field<10, __bond_dummy> : ::bond::detail::mpl::identity<var::st1> {};
        template <int __bond_dummy> struct field<11, __bond_dummy> : ::bond::detail::mpl::identity<var::na> {};
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("foo", "test.foo",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    //
    // withFoo
    //
    struct withFoo::Schema
    {
        typedef ::bond::no_base base;

        static const ::bond::Metadata metadata;
        
        private: static const ::bond::Metadata s_f_metadata;
        private: static const ::bond::Metadata s_f1_metadata;

        public: struct var
        {
            // f
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                withFoo,
                ::test::foo,
                &withFoo::f,
                &s_f_metadata
            > {} f;
        
            // f1
            typedef struct : ::bond::reflection::FieldTemplate<
                1,
                1,
                ::bond::reflection::optional_field_modifier,
                withFoo,
                ::test::foo,
                &withFoo::f1,
                &s_f1_metadata
            > {} f1;
        };

        using field_count = std::integral_constant<uint16_t, 2>;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4348) // VC bug: redefinition of default parameter
#endif
        template <uint16_t I, int = 0> struct field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<var::f> {};
        template <int __bond_dummy> struct field<1, __bond_dummy> : ::bond::detail::mpl::identity<var::f1> {};
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("withFoo", "test.withFoo",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    
} // namespace test

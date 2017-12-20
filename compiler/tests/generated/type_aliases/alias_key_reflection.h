
#pragma once

#include "alias_key_types.h"
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
        
        private: static const ::bond::Metadata s_m_metadata;
        private: static const ::bond::Metadata s_s_metadata;

        public: struct var
        {
            // m
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                foo,
                std::map< ::test::String, ::test::Int, std::less< ::test::String>, typename std::allocator_traits<arena>::template rebind_alloc<std::pair<const ::test::String, ::test::Int> > >,
                &foo::m,
                &s_m_metadata
            > {} m;
        
            // s
            typedef struct : ::bond::reflection::FieldTemplate<
                1,
                1,
                ::bond::reflection::optional_field_modifier,
                foo,
                std::set< ::test::Int, std::less< ::test::Int>, typename std::allocator_traits<arena>::template rebind_alloc< ::test::Int> >,
                &foo::s,
                &s_s_metadata
            > {} s;
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
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<var::m> {};
        template <int __bond_dummy> struct field<1, __bond_dummy> : ::bond::detail::mpl::identity<var::s> {};
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("foo", "test.foo",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    
} // namespace test


#pragma once

#include "field_modifiers_types.h"
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
        
        private: static const ::bond::Metadata s_o_metadata;
        private: static const ::bond::Metadata s_r_metadata;
        private: static const ::bond::Metadata s_ro_metadata;

        public: struct var
        {
            // o
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                Foo,
                bool,
                &Foo::o,
                &s_o_metadata
            > {} o;
        
            // r
            typedef struct : ::bond::reflection::FieldTemplate<
                1,
                1,
                ::bond::reflection::required_field_modifier,
                Foo,
                int16_t,
                &Foo::r,
                &s_r_metadata
            > {} r;
        
            // ro
            typedef struct : ::bond::reflection::FieldTemplate<
                2,
                2,
                ::bond::reflection::required_optional_field_modifier,
                Foo,
                double,
                &Foo::ro,
                &s_ro_metadata
            > {} ro;
        };

        using field_count = std::integral_constant<uint16_t, 3>;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4348) // VC bug: redefinition of default parameter
#endif
        template <uint16_t I, int = 0> struct field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<var::o> {};
        template <int __bond_dummy> struct field<1, __bond_dummy> : ::bond::detail::mpl::identity<var::r> {};
        template <int __bond_dummy> struct field<2, __bond_dummy> : ::bond::detail::mpl::identity<var::ro> {};
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("Foo", "tests.Foo",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    
} // namespace tests

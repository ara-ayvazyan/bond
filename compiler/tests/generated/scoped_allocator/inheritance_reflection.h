
#pragma once

#include "inheritance_types.h"
#include <bond/core/reflection.h>

namespace tests
{
    //
    // Base
    //
    struct Base::Schema
    {
        typedef ::bond::no_base base;

        static const ::bond::Metadata metadata;
        
        private: static const ::bond::Metadata s_x_metadata;

        public: struct var
        {
            // x
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                Base,
                int32_t,
                &Base::x,
                &s_x_metadata
            > {} x;
        };

        using field_count = std::integral_constant<uint16_t, 1>;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4348) // VC bug: redefinition of default parameter
#endif
        template <uint16_t I, int = 0> struct field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<var::x> {};
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("Base", "tests.Base",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    //
    // Foo
    //
    struct Foo::Schema
    {
        typedef ::tests::Base base;

        static const ::bond::Metadata metadata;
        
        private: static const ::bond::Metadata s_x_metadata;

        public: struct var
        {
            // x
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                Foo,
                int32_t,
                &Foo::x,
                &s_x_metadata
            > {} x;
        };

        using field_count = std::integral_constant<uint16_t, 1>;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4348) // VC bug: redefinition of default parameter
#endif
        template <uint16_t I, int = 0> struct field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<var::x> {};
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("Foo", "tests.Foo",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    
} // namespace tests

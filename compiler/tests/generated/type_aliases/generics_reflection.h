
#pragma once

#include "generics_types.h"
#include <bond/core/reflection.h>

namespace tests
{
    //
    // Foo
    //
    template <typename T1, typename T2>
    struct Foo<T1, T2>::Schema
    {
        typedef ::bond::no_base base;

        static const ::bond::Metadata metadata;
        
        private: static const ::bond::Metadata s_t2_metadata;
        private: static const ::bond::Metadata s_n_metadata;

        public: struct var
        {
            // t2
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                Foo<T1, T2>,
                T2,
                &Foo<T1, T2>::t2,
                &s_t2_metadata
            > {} t2;
        
            // n
            typedef struct : ::bond::reflection::FieldTemplate<
                1,
                1,
                ::bond::reflection::optional_field_modifier,
                Foo<T1, T2>,
                ::bond::nullable< ::tests::Foo<T1, bool>, arena>,
                &Foo<T1, T2>::n,
                &s_n_metadata
            > {} n;
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
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<typename var::t2> {};
        template <int __bond_dummy> struct field<1, __bond_dummy> : ::bond::detail::mpl::identity<typename var::n> {};
        
        
        Schema()
        {
            // Force instantiation of template statics
            (void)metadata;
            (void)s_t2_metadata;
            (void)s_n_metadata;
        }
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit<::bond::detail::mpl::list<T1, T2> >("Foo", "tests.Foo",
                ::bond::reflection::Attributes()
            );
        }
    };
    
    template <typename T1, typename T2>
    const ::bond::Metadata Foo<T1, T2>::Schema::metadata
        = Foo<T1, T2>::Schema::GetMetadata();
    
    template <typename T1, typename T2>
    const ::bond::Metadata Foo<T1, T2>::Schema::s_t2_metadata
        = ::bond::reflection::MetadataInit("t2");
    
    template <typename T1, typename T2>
    const ::bond::Metadata Foo<T1, T2>::Schema::s_n_metadata
        = ::bond::reflection::MetadataInit("n");

    
} // namespace tests

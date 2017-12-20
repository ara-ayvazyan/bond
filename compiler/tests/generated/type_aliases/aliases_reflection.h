
#pragma once

#include "aliases_types.h"
#include <bond/core/reflection.h>

namespace tests
{
    //
    // Foo
    //
    template <typename T>
    struct Foo<T>::Schema
    {
        typedef ::bond::no_base base;

        static const ::bond::Metadata metadata;
        
        private: static const ::bond::Metadata s_aa_metadata;

        public: struct var
        {
            // aa
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                Foo<T>,
                ::tests::array10< ::tests::array<20, T> >,
                &Foo<T>::aa,
                &s_aa_metadata
            > {} aa;
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
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<typename var::aa> {};
        
        
        Schema()
        {
            // Force instantiation of template statics
            (void)metadata;
            (void)s_aa_metadata;
        }
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit<::bond::detail::mpl::list<T> >("Foo", "tests.Foo",
                ::bond::reflection::Attributes()
            );
        }
    };
    
    template <typename T>
    const ::bond::Metadata Foo<T>::Schema::metadata
        = Foo<T>::Schema::GetMetadata();
    
    template <typename T>
    const ::bond::Metadata Foo<T>::Schema::s_aa_metadata
        = ::bond::reflection::MetadataInit("aa");

    //
    // WrappingAnEnum
    //
    struct WrappingAnEnum::Schema
    {
        typedef ::bond::no_base base;

        static const ::bond::Metadata metadata;
        
        private: static const ::bond::Metadata s_aWrappedEnum_metadata;

        public: struct var
        {
            // aWrappedEnum
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                WrappingAnEnum,
                ::tests::Wrapper< ::tests::EnumToWrap>,
                &WrappingAnEnum::aWrappedEnum,
                &s_aWrappedEnum_metadata
            > {} aWrappedEnum;
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
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<var::aWrappedEnum> {};
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("WrappingAnEnum", "tests.WrappingAnEnum",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    
} // namespace tests

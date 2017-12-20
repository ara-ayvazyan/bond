
#pragma once

#include "bond_meta_types.h"
#include <bond/core/reflection.h>

namespace deprecated
{
namespace bondmeta
{
    //
    // HasMetaFields
    //
    struct HasMetaFields::Schema
    {
        typedef ::bond::no_base base;

        static const ::bond::Metadata metadata;
        
        private: static const ::bond::Metadata s_full_name_metadata;
        private: static const ::bond::Metadata s_name_metadata;

        public: struct var
        {
            // full_name
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::required_optional_field_modifier,
                HasMetaFields,
                std::string,
                &HasMetaFields::full_name,
                &s_full_name_metadata
            > {} full_name;
        
            // name
            typedef struct : ::bond::reflection::FieldTemplate<
                1,
                1,
                ::bond::reflection::required_optional_field_modifier,
                HasMetaFields,
                std::string,
                &HasMetaFields::name,
                &s_name_metadata
            > {} name;
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
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<var::full_name> {};
        template <int __bond_dummy> struct field<1, __bond_dummy> : ::bond::detail::mpl::identity<var::name> {};
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("HasMetaFields", "deprecated.bondmeta.HasMetaFields",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    
} // namespace bondmeta
} // namespace deprecated
